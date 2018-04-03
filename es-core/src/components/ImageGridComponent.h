#pragma once
#ifndef ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H
#define ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H

#include "components/IList.h"
#include "resources/TextureResource.h"

struct ImageGridData
{
	std::shared_ptr<TextureResource> texture;
};

template<typename T>
class ImageGridComponent : public IList<ImageGridData, T>
{
protected:
	using IList<ImageGridData, T>::mEntries;
	using IList<ImageGridData, T>::listUpdate;
	using IList<ImageGridData, T>::listInput;
	using IList<ImageGridData, T>::listRenderTitleOverlay;
	using IList<ImageGridData, T>::getTransform;
	using IList<ImageGridData, T>::mSize;
	using IList<ImageGridData, T>::mCursor;
	using IList<ImageGridData, T>::Entry;
	using IList<ImageGridData, T>::mWindow;

public:
	using IList<ImageGridData, T>::size;
	using IList<ImageGridData, T>::isScrolling;
	using IList<ImageGridData, T>::stopScrolling;

	ImageGridComponent(Window* window);

	void add(const std::string& name, const std::string& imagePath, const T& obj);
	
	void onSizeChanged() override;

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

private:
	// Calculate how much tiles of size mTileMaxSize we can fit in a grid of size mSize using a margin of size mMargin
	Vector2i calcGridDimension()
	{
		// GRID_SIZE = COLUMNS * TILE_SIZE + (COLUMNS - 1) * MARGIN
		// <=> COLUMNS = (GRID_SIZE + MARGIN) / (TILE_SIZE + MARGIN)
		Vector2f gridDim = (mSize + mMargin) / (mTileMaxSize + mMargin);

		mGridDimension = Vector2i((int) gridDim.x(), (int) gridDim.y());
	};

	void inline setDefaultSelectedTileMaxSize() { mSelectedTileMaxSize = mTileMaxSize * 1.15f; }

	void buildImages();
	void updateImages();

	virtual void onCursorChanged(const CursorState& state);

	bool mEntriesDirty;

	Vector2f mMargin;
	Vector2f mTileMaxSize;
	Vector2f mSelectedTileMaxSize;
	Vector2i mGridDimension;

	std::vector<ImageComponent> mImages;
};

template<typename T>
ImageGridComponent<T>::ImageGridComponent(Window* window) : IList<ImageGridData, T>(window)
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	mEntriesDirty = true;

	mSize = screen * 0.79f;
	mMargin = screen * 0.01f;
	mTileMaxSize = screen * 0.19f;
	setDefaultSelectedTileMaxSize();

	calcGridDimension();
}

template<typename T>
void ImageGridComponent<T>::add(const std::string& name, const std::string& imagePath, const T& obj)
{
	typename IList<ImageGridData, T>::Entry entry;
	entry.name = name;
	entry.object = obj;
	entry.data.texture = ResourceManager::getInstance()->fileExists(imagePath) ? TextureResource::get(imagePath) : TextureResource::get(":/button.png");
	static_cast<IList< ImageGridData, T >*>(this)->add(entry);
	mEntriesDirty = true;
}

template<typename T>
bool ImageGridComponent<T>::input(InputConfig* config, Input input)
{
	if(input.value != 0)
	{
		Vector2i dir = Vector2i::Zero();
		if(config->isMappedTo("up", input))
			dir[1] = -1;
		else if(config->isMappedTo("down", input))
			dir[1] = 1;
		else if(config->isMappedTo("left", input))
			dir[0] = -1;
		else if(config->isMappedTo("right", input))
			dir[0] = 1;

		if(dir != Vector2i::Zero())
		{
			listInput(dir.x() + dir.y() * mGridDimension.x());
			return true;
		}
	}else{
		if(config->isMappedTo("up", input) || config->isMappedTo("down", input) || config->isMappedTo("left", input) || config->isMappedTo("right", input))
		{
			stopScrolling();
		}
	}

	return GuiComponent::input(config, input);
}

template<typename T>
void ImageGridComponent<T>::update(int deltaTime)
{
	listUpdate(deltaTime);
}

template<typename T>
void ImageGridComponent<T>::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = getTransform() * parentTrans;

	if(mEntriesDirty)
	{
		buildImages();
		updateImages();
		mEntriesDirty = false;
	}

	// Dirty solution (took from updateImages function) to keep the selected image and render it later (on top of the others)
	// Will be changed for a cleaner way with the introduction of GridTileComponent
	int cursorRow = mCursor / mGridDimension.x();

	int start = (cursorRow - (mGridDimension.y() / 2)) * mGridDimension.x();

	//if we're at the end put the row as close as we can and no higher
	if(start + (mGridDimension.x() * mGridDimension.y()) >= (int)mEntries.size())
		start = mGridDimension.x() * ((int)mEntries.size()/mGridDimension.x() - mGridDimension.y() + 1);

	if(start < 0)
		start = 0;

	unsigned int i = (unsigned int)start;
	ImageComponent* selectedImage = NULL;
	for(auto it = mImages.begin(); it != mImages.end(); it++)
	{
		// If it's the selected image, keep it for later, otherwise render it now
		if(i == (unsigned int)mCursor)
			selectedImage = it.base();
		else
			it->render(trans);
		i++;
	}

	// Render the selected image on top of the others
	if (selectedImage != NULL)
		selectedImage->render(trans);

	GuiComponent::renderChildren(trans);
}

template<typename T>
void ImageGridComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	GuiComponent::applyTheme(theme, view, element, properties);

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "imagegrid");
	if(!elem)
		return;

	using namespace ThemeFlags;

	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if (elem->has("margin"))
		mMargin = elem->get<Vector2f>("margin") * screen;

	if (elem->has("tileMaxSize"))
		mTileMaxSize = elem->get<Vector2f>("tileMaxSize") * screen;

	if (elem->has("selectedTileMaxSize"))
		mSelectedTileMaxSize = elem->get<Vector2f>("selectedTileMaxSize") * screen;
	else
		setDefaultSelectedTileMaxSize();

	// Recalculate grid dimension after theme changed
	calcGridDimension();
}

template<typename T>
void ImageGridComponent<T>::onCursorChanged(const CursorState& /*state*/)
{
	updateImages();
}

template<typename T>
void ImageGridComponent<T>::onSizeChanged()
{
	buildImages();
	updateImages();
}

// Create and position imagecomponents (mImages)
template<typename T>
void ImageGridComponent<T>::buildImages()
{
	mImages.clear();

	Vector2f startPosition = mTileMaxSize / 2;
	Vector2f tileDistance = mTileMaxSize + mMargin;

	// Layout tile size and position
	for(int y = 0; y < mGridDimension.y(); y++)
	{
		for(int x = 0; x < mGridDimension.x(); x++)
		{
			// Create tiles
			mImages.push_back(ImageComponent(mWindow));
			ImageComponent& image = mImages.at(y * mGridDimension.x() + x);

			image.setPosition(x * tileDistance.x() + startPosition.x(), y * tileDistance.y() + startPosition.y());
			image.setOrigin(0.5f, 0.5f);
			image.setMaxSize(mTileMaxSize);
			image.setImage("");
		}
	}
}

template<typename T>
void ImageGridComponent<T>::updateImages()
{
	if(mImages.empty())
		buildImages();

	int cursorRow = mCursor / mGridDimension.x();

	int start = (cursorRow - (mGridDimension.y() / 2)) * mGridDimension.x();

	// If we are at the end put the row as close as we can and no higher
	// E nb of entries, X grid x dim (nb column), Y grid y dim (nb line)
	// start = first tile of last row - nb column * (nb line - 1)
	//       = (E - 1) / X * X        - X * (Y - 1)
	//       = X * ((E - 1) / X - Y + 1)
	if(start + (mGridDimension.x() * mGridDimension.y()) >= (int)mEntries.size())
		start = mGridDimension.x() * (((int)mEntries.size() - 1) / mGridDimension.x() - mGridDimension.y() + 1);

	if(start < 0)
		start = 0;

	unsigned int i = (unsigned int)start;
	for(unsigned int img = 0; img < mImages.size(); img++)
	{
		ImageComponent& image = mImages.at(img);
		if(i >= (unsigned int)size())
		{
			image.setImage("");
			continue;
		}

		if(i == (unsigned int)mCursor)
		{
			image.setColorShift(0xFFFFFFFF);
			image.setMaxSize(mSelectedTileMaxSize);
		}else{
			image.setColorShift(0xAAAAAABB);
			image.setMaxSize(mTileMaxSize);
		}

		image.setImage(mEntries.at(i).data.texture);
		i++;
	}
}

#endif // ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H
